# tynsel

**tynsel** tries to be a space- and time-efficient software modem allowing data to be transmitted and received by 300-baud [FSK] using the [Bell 103 dataset]. This is an obsolete technology from the 1980s, and is not meant to be used in new projects, but it could in principle be used to enable the use of ancient hardware.

This code is intentionally written to fit in an AVR [ATTINY412] microcontroller and therefore to use less than 256 bytes of RAM and 4KiB of program space. The current implementation builds most of the code for AVR and allows for manual checking that the program size is appropriately small, but the only working implementation runs as a UNIX process which can interact with the physical (audio) world via [SoX]. Someday, the tynsel software might be hosted within a hardware design like [soylent].

[FSK]: https://en.wikipedia.org/wiki/Frequency-shift_keying
[Bell 103 dataset]: https://en.wikipedia.org/wiki/Bell_103_modem
[ATTINY412]: https://www.microchip.com/wwwproducts/en/ATTINY412
[SoX]: http://sox.sourceforge.net
[soylent]: https://github.com/kulp/soylent
