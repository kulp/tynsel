# tynsel

**tynsel** tries to be a space- and time-efficient software modem allowing data to be transmitted and received by 300-baud [FSK] using the [Bell 103 dataset]. This is an obsolete technology from the 1980s, and is not meant to be used in new projects, but it could in principle be used to enable the use of ancient hardware.

**Before you use tynsel, look at [minimodem] instead; it is much more capable software than tynsel.**

## Rationale

This code is intentionally written to fit in an AVR [ATTINY412] microcontroller and therefore to use less than 256 bytes of RAM and 4KiB of program space.

The (non-working) implementation in the `avr-top` binary allows for manual checking that the program size is appropriately small. The (working) UNIX implementation in the `gen` and `listen` binaries can interact with the physical (audio) world via tools like [SoX], as shown in examples below.

Someday, the tynsel software might be hosted within a hardware design like [soylent].

## Building

In order to compute the notch filter coefficients, you will need the `signal` package installed for [GNU Octave]:

    octave --eval "pkg install -forge signal control"

Unless you are building for an AVR target like the [ATTINY412], you probably want to build just the `gen` and `listen` targets:

    make gen listen

## Basic functionality

The `gen` binary takes ASCII data on `stdin` and produces raw monoaural audio on `stdout` as 16-bit signed integers at 8000Hz by default. The `listen` binary does the reverse, so they can be chained together, as illustrated in the examples below.

Until more documentation is written, you can get an idea of the available options by reviewing the `parse_opts` functions in `src/gen.c` and `src/listen.c`.

## Example usage

The following command will emit "hello":

    echo "hello" | ./gen | ./listen

You can listen to the output using [SoX]'s `play` command:

    echo "hello, world" | ./gen |
        play --rate 8000 --encoding signed --bits 16 --type raw -

By default, `gen` produces produces only enough samples to represent its input, plus a bit of carrier padding at the beginning and end of transmission.

You can use `gen` to generate data in real time, using the `-r 1` option; in this case, input received from the keyboard will be translated and played out your default sound device:

    ./gen -r 1 |
        play --rate 8000 --encoding signed --bits 16 --type raw --no-show-progress -

### Interoperating with [minimodem]

Sending from [minimodem] and receiving in tynsel:

    echo 'hello from minimodem' |
        minimodem --tx 300 --volume 0.5 --file minimodem.wav --samplerate 8000 --stopbits 2
    sox minimodem.wav --encoding signed --bits 16 --type raw - |
        ./listen -C 0

Sending from tynsel and receiving in [minimodem]:

    echo 'hello from tynsel' |
        ./gen -C 0 |
        sox --rate 8000 --encoding signed --bits 16 --type raw - gen.wav
    minimodem --rx 300 --file gen.wav

[FSK]: https://en.wikipedia.org/wiki/Frequency-shift_keying
[Bell 103 dataset]: https://en.wikipedia.org/wiki/Bell_103_modem
[ATTINY412]: https://www.microchip.com/wwwproducts/en/ATTINY412
[SoX]: http://sox.sourceforge.net
[soylent]: https://github.com/kulp/soylent
[minimodem]: http://www.whence.com/minimodem/
[GNU Octave]: https://octave.org
