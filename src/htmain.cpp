#include "opentxbt.h"
#include "bluetooth.h"

static uint16_t channel_data[BT_CHANNELS];
static uint16_t bt_chans[BT_CHANNELS];

void calculate_Thread()
{
    for (int i = 0; i < BT_CHANNELS; i++)
      bt_chans[i] = 0;  // Reset all BT in channels to Zero (Not active)
    for (int i = 0; i < BT_CHANNELS; i++) {
      uint16_t btvalue = btGetChannel(i);
      if (btvalue > 0) {
        bt_chans[i] = btvalue;
        channel_data[i] = btvalue;
      }
    }

}

