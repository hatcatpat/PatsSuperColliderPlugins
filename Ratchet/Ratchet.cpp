#include "SC_PlugIn.hpp"

static uint negmod(int x, uint n)
{
    return ((x % n) + n) % n;
}

static InterfaceTable *ft;

struct Ratchet : public SCUnit
{
public:
    Ratchet()
    {
        bufsize = NEXTPOWEROFTWO((float)sampleRate() * max_bufsize_s);
        bufsize_minus_1 = bufsize - 1;

        record_buf = (float *)RTAlloc(mWorld, bufsize * sizeof(float));
        playback_buf = (float *)RTAlloc(mWorld, bufsize * sizeof(float));

        memset(record_buf, 0, bufsize * sizeof(float));
        memset(playback_buf, 0, bufsize * sizeof(float));

        if (record_buf == NULL || playback_buf == NULL)
        {
            if (mWorld->mVerbosity > -2)
            {
                Print("Failed to allocate memory for Ratchet ugen.\n");
            }

            return;
        }

        set_calc_function<Ratchet, &Ratchet::next>();
        next(1);
    }

    ~Ratchet()
    {
        RTFree(mWorld, record_buf);
        RTFree(mWorld, playback_buf);
    }

private:
    const float max_bufsize_s{1.0f};

    int bufsize;
    int bufsize_minus_1;

    float *record_buf;
    float *playback_buf;

    int record_pos{0};
    float playback_pos{0.0f};

    int loop_end_sample{0};

    bool playing{false};

private:
    void next(int inNumSamples)
    {
        const float *input = in(0);
        float *outbuf = out(0);
        const bool playing_param = in0(1) > 0;
        float length_param = in0(2);
        if (length_param < 0)
            length_param = -length_param;
        if (length_param > 1)
            length_param = 1;
        const float playback_rate_param = in0(3);

        const int loop_start_sample = negmod((int)floor(loop_end_sample - bufsize * length_param), bufsize);
        const bool simple_loop = loop_start_sample < loop_end_sample;
        const bool forward = playback_rate_param > 0.0f;

        if (playing != playing_param)
        {
            if (playing_param)
            {
                begin_playback();
            }
            else
            {
                playing = false;
            }
        }

        for (int i = 0; i < inNumSamples; i++)
        {
            record_buf[record_pos] = input[i];
            record_pos = (record_pos + 1) & bufsize_minus_1;

            if (playing)
            {
                outbuf[i] = playback_buf[(int)floor(playback_pos)];

                playback_pos += playback_rate_param;
                if (playback_pos >= bufsize)
                    playback_pos = 0;
                else if (playback_pos < 0)
                    playback_pos = bufsize - 1;

                if (simple_loop)
                {
                    if (playback_pos >= loop_end_sample)
                        playback_pos = loop_start_sample;
                    else if (playback_pos < loop_start_sample)
                        playback_pos = loop_end_sample - 1;
                }
                else
                {
                    if (playback_pos >= loop_end_sample && playback_pos < loop_start_sample)
                        playback_pos = forward ? loop_start_sample : loop_end_sample - 1;
                }
            }
            else
            {
                outbuf[i] = input[i];
            }
        }
    }

    void begin_playback()
    {
        for (int i = 0; i < bufsize; i++)
        {
            playback_buf[i] = record_buf[i];
        }
        loop_end_sample = record_pos;
        playing = true;
    }
};

PluginLoad(RatchetUGens)
{
    ft = inTable;
    registerUnit<Ratchet>(ft, "Ratchet");
}