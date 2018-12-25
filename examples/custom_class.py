import sys
from typing import List

from saleae_enrichable_spi_analyzer import (
    EnrichableSpiAnalyzer, Channel, Marker, MarkerType
)


class SC16IS7xxAnalyzer(EnrichableSpiAnalyzer):
    ENABLE_MARKER = False

    CHANNEL_NAMES = {
        0b00: 'A',
        0b01: 'B',
    }

    REGISTER_NAMES = {
        0x00: ('RHR / DLL(9)', 'THR / DLL(9)'),
        0x01: 'IER',
        0x02: ('IIR / EFR(10)', 'FCR / EFR(10)'),
        0x03: 'LCR',
        0x04: 'MCR / XON1(10)',
        0x05: 'LSR / XON2(10)',
        0x06: 'MSR / TCR(6) / XOFF1(10)',
        0x07: 'SPR / TLR(6) / XOFF2(10)',
        0x08: 'TXLVL',
        0x09: 'RXLVL',
        0x0A: 'IODir',
        0x0B: 'IOState',
        0x0C: 'IOIntEna',
        0x0D: '<Reserved>',
        0x0E: 'IOControl',
        0x0F: 'EFCR',
    }
    # (6): These are accessible only when EFR[4] = logic 1, and MCR[2] = logic 1
    # (9): These are accessible only when LCR[7] = logic 1, and LCR is not 0xBF
    # (10): These are acessible only when LCR is 0xBF

    def get_register_name(self, id, read):
        register_name_data = self.REGISTER_NAMES[id]

        if isinstance(register_name_data, str):
            return register_name_data
        else:
            return register_name_data[0] if read else register_name_data[1]

    def __init__(self, *args, **kwargs):
        super(SC16IS7xxAnalyzer, self).__init__(*args, **kwargs)
        self.request_phase = False
        self.request_is_write = False

    def get_bubble_text(
        self,
        frame_index: int,
        start_sample: int,
        end_sample: int,
        frame_type: int,
        flags: int,
        direction: Channel,
        value: int
    ) -> str:
        if direction == Channel.MOSI:
            if not self.request_phase:
                self.request_phase = True

                read = value & 0x80
                self.request_is_write = not read
                register = (value >> 3) & 0xf
                channel = (value >> 1) & 0x3

                if channel not in self.CHANNEL_NAMES:
                    # If we found an unexpected channel, we're probably
                    # mis-guessing which phase we're in.
                    self.request_phase = False
                    return

                return (
                    "({readwrite}) {register} Ch {channel}".format(
                        readwrite="R" if read else "W",
                        register=self.get_register_name(register, read),
                        channel=self.CHANNEL_NAMES[channel]
                    )
                )
            else:
                self.request_phase = False
                if self.request_is_write:
                    return hex(value)
        else:
            if not self.request_phase and not self.request_is_write:
                return hex(value)

    def get_markers(
        self,
        frame_index: int,
        sample_count: int,
        start_sample: int,
        end_sample: int,
        frame_type: int,
        flags: int,
        mosi_value: int,
        miso_value: int
    ) -> List[Marker]:
        return []


if __name__ == '__main__':
    SC16IS7xxAnalyzer.run(sys.argv[1:])
