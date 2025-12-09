int encodeSigned5(int value) {
    // make sure value fits in 5 bits
    if (value < -16 || value > 15) {
        cerr << "Offset out of 5-bit signed range\n";
        return 0;
    }

    // make it a 5-bit two's complement
    if (value < 0)
        value = (1 << 5) + value;  // add 32 to wrap

    return value & 0x1F;  // keep only 5 bits
}
