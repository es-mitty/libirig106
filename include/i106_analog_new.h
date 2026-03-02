extern "C" {
    #include "i106_analog.h"
}

#include <unordered_map>
#include <vector>

/** Summary
 * 
 * Analog decoding is complicated, but here is my general understanding of it:
 * 
 * Each analog packet consists of channels and subchannels, where a channel can typically be thought of as
 * a physical ADC which samples its inputs (sub-channels) at different rates. One analog packet can contain multiple readings
 * from multiple sub-channels, and the sample rate of each sub-channel determines the order in which those readings
 * are packed into the packet (assuming we are in packed mode).
 * 
 * More on those sample rates I just mentioned: 
 * 
 * In each analog packet, there are multiple channel specific data words (CSDWs)
 * that describe each sub channel. They contain the number of sub-channels present in the packet. They also contain a fraction of
 * the packet-wide sample rate, which indicates how many times the measurement occurs in relation to the packet-wide rate. The packet-wide
 * rate is defined in the TMATs file, and really only tells you how to correlate each sample to a real-world time. Without knowing it, you
 * can still determine the order of the samples in the analog packet. Here is the generic way to do that:
 * 
 * Generic determination of sample schedule within a single packet:
 * 
 * Read each CSDW (one per sub-channel present in the packet) and grab the sample factor. Iterate through the packet payload until there
 * are no bytes left, and do the following while you iterate:
 *     Keep track of a "tick" value. The tick is how many times you have iterated from the start of the packet.
 *     For each sub-channel (in order of the CSDW occurence) see if the tick is a multiple of the sample factor for that sub-channel.
 *          If it is, expect to see it...right now. Decode it using the number of bits in the CSDW (packed mode only)
 * 
 * Since the number of total samples in a packet cannot be predetermined, this will require some dynamically allocated buffers...OR maybe it
 * can be determined based on the packet length and sample factors, I don't know yet.
 * 
 * AND ALSO, if the CSDW indicates "Same" mode, then ignore all of this. The readings are all from the same sub-channel lol.
 */

struct I106_AnalogF1_Subchannel_Sample {
    uint8_t subchannel;
    uint32_t occurence_in_pkt;
    uint32_t value;
};

struct I106_Analog_F1_Decoded_Packet {
    I106C10Header header;
    std::unordered_map<uint8_t, AnalogF1_CSDW> subchannel_csdws;
    std::unordered_map<uint8_t, std::vector<uint8_t>> channels_sampled_per_factor_value;
    uint32_t highest_sample_factor;
    uint32_t sample_rate_denominator;
    uint32_t num_samples;
    std::vector<I106_AnalogF1_Subchannel_Sample> samples;
};

I106Status I106_Decode_Raw_AnalogF1(I106C10Header *header, uint8_t *buffer, I106_Analog_F1_Decoded_Packet *decoded_packet_handle){
    // Decode the first csdw to determine how many more csdws there are
    int bytes_read = 0;
    decoded_packet_handle->header = *header;
    AnalogF1_CSDW *first_csdw = (AnalogF1_CSDW *)buffer;
    bytes_read += sizeof(AnalogF1_CSDW);
    uint8_t num_subchannels = first_csdw->Subchannels;
    decoded_packet_handle->subchannel_csdws[first_csdw->Subchannel] = *first_csdw;
    
    // Grab other csdws
    for (int i = 1; i < num_subchannels; i++){
        AnalogF1_CSDW *csdw = (AnalogF1_CSDW *)((char *)buffer + bytes_read);
        decoded_packet_handle->subchannel_csdws[csdw->Subchannel] = *csdw;
        bytes_read += sizeof(AnalogF1_CSDW);
    }

    // Populate channels_sampled_per_factor_value and highest_sample_factor
    decoded_packet_handle->highest_sample_factor = 0;

    // normalize factors (0 -> 1), find highest factor, and populate channels_sampled_per_factor_value
    for (auto &[subchannel, csdw] : decoded_packet_handle->subchannel_csdws) {
        if (csdw.Factor == 0) {
            csdw.Factor = 1;
        }
        if (csdw.Factor > decoded_packet_handle->highest_sample_factor) {
            decoded_packet_handle->highest_sample_factor = csdw.Factor;
        }
        // Ensure that all lower factor values have this channel as well.
        for (int factor = csdw.Factor; factor > 0; factor /= 2) {
            decoded_packet_handle->channels_sampled_per_factor_value[factor].push_back(csdw.Subchannel);
        }
    }

    // While payload bytes remain
    uint32_t tick = 0;
    int bits_read = 0;
    while (bits_read / 8 < header->DataLength){
        // Determine 
        int highest_factor_for_tick = 0;
        for (int i = decoded_packet_handle->highest_sample_factor; i > 0; i/=2){
            if (tick % i == 0 && decoded_packet_handle->channels_sampled_per_factor_value.contains(i)){
                highest_factor_for_tick = i;
                break;
            }
        }
        // Get the channels that are sampled at this factor
        if (decoded_packet_handle->channels_sampled_per_factor_value.contains(highest_factor_for_tick)){
            std::vector<uint8_t> channels_sampled_at_this_factor = decoded_packet_handle->channels_sampled_per_factor_value[highest_factor_for_tick];
            for (uint8_t channel : channels_sampled_at_this_factor){
                // Decode the sample value for this channel
                uint32_t sample_value = 0;
                int num_bits = decoded_packet_handle->subchannel_csdws[channel].Length;
                uint32_t sample_factor = decoded_packet_handle->subchannel_csdws[channel].Factor;
                for (int i = 0; i < num_bits; i++){
                    int byte_index = (bits_read + i) / 8;
                    int bit_index = (bits_read + i) % 8;
                    uint8_t bit = (buffer[bytes_read + byte_index] >> (7 - bit_index)) & 1;
                    sample_value = (sample_value << 1) | bit;
                }
                bits_read += num_bits;
                decoded_packet_handle->samples.push_back({channel, tick / sample_factor, sample_value});
            }
        }
        tick++;
    }
    bytes_read += (bits_read + 7) / 8; // Round up to nearest byte
    buffer += bytes_read;
    return I106_OK;
}