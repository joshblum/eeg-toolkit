#ifndef CONFIG_H
#define CONFIG_H
#include<string>

using namespace std;

// backend config
#ifndef BACKEND
#define BACKEND BinaryBackend
#endif
// Helpers to print string name of backend
#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

// Path to EEG data
#ifndef DATADIR
#define DATADIR "/home/ubuntu/eeg-data/eeg-data/"
#endif

// Max size to read
#ifndef READ_CHUNK
#define READ_CHUNK 4
#endif
#define READ_CHUNK_SIZE (1000000 * READ_CHUNK) // MB

// Max size to write
#ifndef WRITE_CHUNK
#define WRITE_CHUNK 4
#endif
#define WRITE_CHUNK_SIZE (1000000 * WRITE_CHUNK) // MB

// websocket server config
#define WS_DEFAULT_PORT 8080

// spectrogram config
typedef enum
{
  LL = 0,
  LP,
  RP,
  RL
} ch_t;

// map channel names to index in EDF
typedef enum
{
  C3 = 0,
  C4 = 1,
  O1 = 2,
  O2 = 3,
  F3 = 7,
  F4 = 8,
  F7 = 9,
  F8 = 10,
  FP1 = 12,
  FP2 = 13,
  P3 = 15,
  P4 = 16,
  T3 = 18,
  T4 = 19,
  T5 = 20,
  T6 = 21,
  ALL = 22,
} ch_idx_t;

static const int NCHANNELS = 16;
static const int CHANNEL_ARRAY[NCHANNELS] = {
  C3,
  C4,
  O1,
  O2,
  F3,
  F4,
  F7,
  F8,
  FP1,
  FP2,
  P3,
  P4,
  T3,
  T4,
  T5,
  T6
};

// EDF files store data in the channels given in `ch_idx_t`
// Other array stores won't skip columns though, so we use
// this table to convert a channel (e.g. T6 = 21) to an
// index in a different array store.
static const int CH_REVERSE_IDX[23] = {
  0, // C3 = 0,
  1, // C4 = 1,
  2, // O1 = 2,
  3, // O2 = 3,
  -1, // noop 4
  -1, // noop 5
  -1, // noop 6
  4, // F3 = 7,
  5, // F4 = 8,
  6, // F7 = 9,
  7, // F8 = 10,
  -1, // noop 11
  8, // FP1 = 12,
  9, // FP2 = 13,
  -1, // noop 14
  10, // P3 = 15,
  11, // P4 = 16,
  -1, // noop 17
  12, // T3 = 18,
  13, // T4 = 19,
  14, // T5 = 20,
  15, // T6 = 21,
  0, // ALL = 22,
};

static const int NUM_DIFFS = 5;
typedef struct ch_diff
{
  ch_t ch; // channel name
  // store an array of channels used
  // in the diff. The diff is computed by
  // subtracting channels: (i + 1) - (i)
  ch_idx_t ch_idx[NUM_DIFFS];
} ch_diff_t;

// Brain regions for EEG
static const int NUM_DIFF = 4;
const ch_diff_t DIFFERENCE_PAIRS[NUM_DIFF] =
{
  // (FP1 - F7)
  // (F7 - T3)
  // (T3 - T5)
  // (T5 - O1)
  {.ch = LL, .ch_idx = {FP1, F7, T3, T5, O1}},

  // (FP2 - F3)
  // (F3 - C3)
  // (C3 - P3)
  // (P3 - O1)
  {.ch = LP, .ch_idx = {FP1, F3, C3, P3, O1}},

  // (FP2 - F4)
  // (F4 - C4)
  // (C4 - P4)
  // (P4 - O2)
  {.ch = RP, .ch_idx = {FP2, F4, C4, P4, O2}},

  // (FP2 - F8)
  // (F8 - T4)
  // (T4 - T6)
  // (T6 - O2)
  {.ch = RL, .ch_idx = {FP2, F8, T4, T6, O2}},
};

// String names for brain regions
static const string CH_NAME_MAP[NUM_DIFF] = {"LL", "LP", "RP", "RL"};

#endif // CONFIG_H
