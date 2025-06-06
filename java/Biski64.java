import java.math.BigInteger;

/**
 * Biski64 is a high-performance pseudo-random number generator (PRNG)
 * with a guaranteed minimum period of 2^64. It is designed for
 * non-cryptographic applications where speed and statistical quality are important.
 * <p>
 * **Important Note on Parallel Usage:** Instances of this class are NOT thread-safe.
 * To use this generator in a multi-threaded environment, each thread must have its own
 * distinct instance of {@code Biski64}, initialized as a unique stream.
 */
public class Biski64 {
    protected long mix;
    protected long loopMix;
    protected long fastLoop;

    /**
     * Creates a new Biski64 generator for a single stream. The seed is
     * initialized using a value derived from {@code System.nanoTime()}.
     */
    public Biski64() {
        this(System.nanoTime());
    }


    /**
     * Creates a new Biski64 generator for a single stream using a specific seed.
     *
     * @param seed the initial seed
     */
    public Biski64(long seed) {
        setSeed(seed);
    }


    /**
     * Creates a new Biski64 generator for a given stream in a multi-stream setup.
     * A seed is generated automatically.
     *
     * @param streamIndex     the index of the stream (from 0 to totalNumStreams-1)
     * @param totalNumStreams the total number of streams
     * @throws IllegalArgumentException if stream parameters are invalid.
     */
    public Biski64(int streamIndex, int totalNumStreams) {
        // Generate a robust initial seed that differs for each stream.
        long initialSeed = System.nanoTime() ^ Thread.currentThread().threadId() ^ (((long) streamIndex << 32) | totalNumStreams);
        setSeedForStream(initialSeed, streamIndex, totalNumStreams);
    }


    /**
     * Creates a new Biski64 generator for a given stream using a specific seed.
     *
     * @param seed            the initial seed for all streams
     * @param streamIndex     the index of the stream (from 0 to totalNumStreams-1)
     * @param totalNumStreams the total number of streams
     * @throws IllegalArgumentException if stream parameters are invalid.
     */
    public Biski64(long seed, int streamIndex, int totalNumStreams) {
        setSeedForStream(seed, streamIndex, totalNumStreams);
    }


    /**
     * A private helper to warm up the generator by cycling it a few times.
     */
    private void warmup() {
        for (int i = 0; i < 16; i++) {
            nextLong();
        }
    }
    

    /**
     * Reseeds this generator for use as a single stream. The three internal
     * state variables are derived from the given seed.
     *
     * @param seed the initial seed
     */
    public void setSeed(long seed) {
        long seederState = seed;
        this.mix = splitMix64(seederState);

        seederState = this.mix; // Use output to seed the next state for better mixing
        this.loopMix = splitMix64(seederState);

        seederState = this.loopMix;
        this.fastLoop = splitMix64(seederState);

        warmup();
    }


    /**
     * Initializes or reseeds this generator for a specific stream in a
     * multi-stream environment. This method ensures each stream will produce a
     * unique, non-overlapping sequence of numbers.
     *
     * @param seed            The base seed for all streams.
     * @param streamIndex     The index of this specific stream.
     * @param totalNumStreams The total number of streams being used.
     * @throws IllegalArgumentException if stream parameters are invalid.
     */
    public void setSeedForStream(long seed, int streamIndex, int totalNumStreams) {
        if (totalNumStreams < 1) {
            throw new IllegalArgumentException("Total number of streams must be at least 1.");
        }
        if (streamIndex < 0 || streamIndex >= totalNumStreams) {
            throw new IllegalArgumentException("Stream index must be between 0 and totalNumStreams-1.");
        }

        // Initialize mix and loopMix from the base seed.
        long seederState = seed;
        this.mix = splitMix64(seederState);
        seederState = this.mix;

        this.loopMix = splitMix64(seederState);
        seederState = this.loopMix;

        long baseFastLoop = splitMix64(seederState);
        if (totalNumStreams > 1) {
            // Unsigned 2^64-1
            final BigInteger ULONG_MAX = new BigInteger("FFFFFFFFFFFFFFFF", 16);

            // Represent the increment as a BigInteger to ensure correct multiplication.
            final BigInteger WEYL_BIG = new BigInteger("9999999999999999", 16);

            // Calculate how many cycles are in each stream's block.
            BigInteger cyclesPerStream = ULONG_MAX.divide(BigInteger.valueOf(totalNumStreams));

            // Calculate the state offset for this stream: streamIndex * cyclesPerStream * 0x9999999999999999L;
            BigInteger offset = BigInteger.valueOf(streamIndex)
                .multiply(cyclesPerStream)
                .multiply(WEYL_BIG);

            // Add the offset to the random base value. The final .longValue() correctly
            // truncates to 64 bits, which is equivalent to modular arithmetic.
            this.fastLoop = BigInteger.valueOf(baseFastLoop).add(offset).longValue();
        } else {
            // If there's only one stream, no offset is needed.
            this.fastLoop = baseFastLoop;
        }
        
    warmup();
    }


    /**
     * A SplitMix64 helper function to scramble and distribute seed bits.
     *
     * @param z The input value.
     * @return A pseudo-random long derived from the input.
     */
    public static long splitMix64(long z) {
        z = (z ^ (z >>> 30)) * 0xbf58476d1ce4e5b9L;
        z = (z ^ (z >>> 27)) * 0x94d049bb133111ebL;
        return z ^ (z >>> 31);
    }


    /**
     * Returns the next pseudorandom {@code long} value from this generator's sequence.
     * The general contract of {@code nextLong} is that one {@code long}
     * value is pseudorandomly generated and returned.
     *
     * @return the next pseudorandom {@code long} value
     */
    public long nextLong() {
        final long output = this.mix + this.loopMix;
        final long oldLoopMix = this.loopMix;

        this.loopMix = this.fastLoop ^ this.mix;
        this.mix = Long.rotateLeft(this.mix, 16) + Long.rotateLeft(oldLoopMix, 40);
        this.fastLoop += 0x9999999999999999L;

        return output;
    }


    /**
     * Returns a pseudorandom {@code double} value between 0.0 (inclusive) and 1.0 (exclusive).
     *
     * @return a pseudorandom {@code double} value between 0.0 and 1.0
     */
    public double random() {
        // Uses the top 53 bits from nextLong() for double precision
        return (nextLong() >>> 11) * (1.0 / (1L << 53));
    }


    /**
     * Returns a pseudorandom {@code int} value between 0 (inclusive)
     * and the specified value (inclusive), without modulo bias.
     *
     * @param maxInclusive the maximum inclusive bound. Must be non-negative.
     * @return a pseudorandom {@code int} value between 0 and {@code maxInclusive} (inclusive)
     * @throws IllegalArgumentException if {@code maxInclusive} is negative
     */
    public int random(int maxInclusive) {
        if (maxInclusive < 0) {
            throw new IllegalArgumentException("maxInclusive must be non-negative");
        }
        if (maxInclusive == 0) {
            return 0;
        }
        long N = (long) maxInclusive + 1;
        long bits;
        long val;
        do {
            bits = nextLong() & Long.MAX_VALUE; // Ensure non-negative
            val = bits % N;
        }
        while (bits - val + (N - 1) < 0); // Debiasing loop

        return (int) val;
    }


    /**
     * Returns a pseudorandom {@code boolean} value.
     *
     * @return a pseudorandom {@code boolean} value
     */
    public boolean flipCoin() {
        return (nextLong() & 1L) == 0L;
    }


    /**
     * Returns a pseudorandom {@code double} value selected from a
     * Gaussian (normal) distribution with mean 0.0 and standard deviation 1.0.
     *
     * @return a pseudorandom {@code double} from a standard normal distribution
     */
    public double randomGaussian() {
        // Uses the Box-Muller transform
        double v1, v2, s;
        do {
            v1 = 2.0 * random() - 1.0; // Between -1.0 and 1.0
            v2 = 2.0 * random() - 1.0; // Between -1.0 and 1.0
            s = v1 * v1 + v2 * v2;
        }
        while (s >= 1.0 || s == 0.0);

        double multiplier = Math.sqrt((-2.0 * Math.log(s)) / s);

        return v1 * multiplier;
        // Note: v2 * multiplier would be the second Gaussian random number, discarded here.
    }


    /**
     * Returns a pseudorandom hexadecimal string of the specified length.
     * Characters are from '0'-'9' and 'a'-'f'.
     *
     * @param length the desired length of the hex string.
     * @return a random hex string of the specified length.
     */
    public String randomHexString(int length) {
        if (length < 0) {
            throw new IllegalArgumentException("length must be non-negative");
        }
        if (length == 0) {
            return "";
        }
        StringBuilder sb = new StringBuilder(length);
        while (sb.length() < length) {
            // Generate a random long
            long r = nextLong();

            // toHexString will generate up to 16 hex digits
            String hex = String.format("%016x", r);
            sb.append(hex);
        }

        // Ensure exact length, trim if necessary.
        return sb.substring(0, length);
    }


    public static void main(String[] args) {
        System.out.println("--- Single Stream Demonstration ---");
        // Create a generator with a fixed seed for repeatable results.
        Biski64 rng = new Biski64(12345L);
        System.out.println("5 random longs from a single stream (Seed: 12345L):");
        for (int i = 0; i < 5; i++) {
            System.out.println("  " + rng.nextLong());
        }

        System.out.println("\n--- Multi-Stream Demonstration ---");
        final long sharedSeed = 67890L;
        final int numStreams = 4;
        System.out.printf("Generating the first value from %d parallel streams (Shared Seed: %dL):\n", numStreams, sharedSeed);

        Biski64[] streams = new Biski64[numStreams];
        for (int i = 0; i < numStreams; i++) {
            streams[i] = new Biski64(sharedSeed, i, numStreams);
            // The first value from each stream should be different.
            System.out.printf("  Stream %d: %d\n", i, streams[i].nextLong());
        }

        System.out.println("\nTo show the streams continue independently:");
        // Generate another number from the first stream.
        System.out.printf("  Next value from Stream 0: %d\n", streams[0].nextLong());
        // Generate another number from the second stream.
        System.out.printf("  Next value from Stream 1: %d\n", streams[1].nextLong());
    }
}
