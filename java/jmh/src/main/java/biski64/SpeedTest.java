package biski64;

import org.openjdk.jmh.annotations.*;
import org.openjdk.jmh.infra.Blackhole;

import java.util.Random;
import java.util.SplittableRandom; // Added import
import java.util.concurrent.ThreadLocalRandom;
import java.util.concurrent.TimeUnit;
import java.util.random.RandomGenerator;
import java.util.random.RandomGeneratorFactory;

@BenchmarkMode(Mode.AverageTime)
@OutputTimeUnit(TimeUnit.NANOSECONDS)
@State(Scope.Benchmark)
@Warmup(iterations = 5, time = 1, timeUnit = TimeUnit.SECONDS)
@Measurement(iterations = 5, time = 1, timeUnit = TimeUnit.SECONDS)
@Fork(value = 1, jvmArgsAppend = { "-Xms2g", "-Xmx2g" })
public class SpeedTest {

    // Define GR constant locally, common 64-bit Golden Ratio.
    // Used by XoroshiroState.
    private static final long GR = 0x9E3779B97F4A7C15L;

    // State for Java's built-in Random.
    @State(Scope.Thread)
    public static class JavaRandomState {
        public Random random = new Random(System.nanoTime() + Thread.currentThread().getId());
    }

    // State for SplittableRandom.
    @State(Scope.Thread)
    public static class SplittableRandomState {
        public SplittableRandom splittableRandom;

        @Setup(Level.Trial)
        public void init() {
            // Initialize SplittableRandom with a seed.
            // Using nanoTime + threadId for a simple unique seed per thread.
            splittableRandom = new SplittableRandom(System.nanoTime() + Thread.currentThread().getId());
        }
    }

    // State for Xoroshiro128++.
    @State(Scope.Thread)
    public static class XoroshiroState {
        public long s0;
        public long s1;

        @Setup(Level.Trial)
        public void init() {
            long threadIdBits = Thread.currentThread().getId() << 32;
            s0 = (GR * GR) ^ threadIdBits;
            s1 = GR ^ (threadIdBits >>> 16);

            // "Warm up" the xoroshiro state a bit
            for (int i = 0; i < 10; i++) {
                long r = Long.rotateLeft(s0 + s1, 17) + s0; // This line is just to use the result to avoid dead code elimination if it were a real usage
                long tempS1 = s1 ^ s0;
                s0 = Long.rotateLeft(s0, 49) ^ tempS1 ^ (tempS1 << 21);
                s1 = Long.rotateLeft(tempS1, 28);
            }
        }
    }

    /**
     * Internal class within SpeedTest for biski64 state.
     */
    @State(Scope.Thread)
    public static class BiskiState {
        public long mix;
        public long loopMix;
        public long fastLoop;

        @Setup(Level.Trial)
        public void init() {
            long seed = System.nanoTime() + Thread.currentThread().getId();
            Random seeder = new Random(seed);
            mix = seeder.nextLong();
            loopMix = seeder.nextLong();
            fastLoop = seeder.nextLong();

            // "Warm up" the biski64 state
            for (int i = 0; i < 20; i++) {
                long ignoredOutput = this.mix + this.loopMix; // Use the value to prevent potential dead code elimination
                long oldLoopMixState = this.loopMix;
                this.loopMix = this.fastLoop ^ this.mix;
                this.mix = Long.rotateLeft(this.mix, 16) + Long.rotateLeft(oldLoopMixState, 40);
                this.fastLoop += 0x9999999999999999L;
            }
        }
    }

    // State for Xoshiro256++.
    @State(Scope.Thread)
    public static class Xoshiro256State {
        public long s0, s1, s2, s3;

        private static long splitMix64(long x) {
            x += 0x9e3779b97f4a7c15L;
            x = (x ^ (x >>> 30)) * 0xbf58476d1ce4e5b9L;
            x = (x ^ (x >>> 27)) * 0x94d049bb133111ebL;
            return x ^ (x >>> 31);
        }

        @Setup(Level.Trial)
        public void init() {
            long seed = System.nanoTime() + Thread.currentThread().getId();
            s0 = splitMix64(seed);
            s1 = splitMix64(s0);
            s2 = splitMix64(s1);
            s3 = splitMix64(s2);

            // "Warm up" the xoshiro256++ state
            for (int i = 0; i < 20; i++) {
                final long result = Long.rotateLeft(s1 * 5, 7) * 9; // Use the value
                final long t = s1 << 17;
                s2 ^= s0;
                s3 ^= s1;
                s1 ^= s2;
                s0 ^= s3;
                s2 ^= t;
                s3 = Long.rotateLeft(s3, 45);
            }
        }
    }

    // State for Java 17+ RandomGenerator (L64X128MixRandom)
    @State(Scope.Thread)
    public static class RandomGeneratorState {
        public RandomGenerator randomGenerator;

        @Setup(Level.Trial)
        public void init() {
            long seed = System.nanoTime() + Thread.currentThread().getId();
            try {
                // Try to get the specific generator
                randomGenerator = RandomGeneratorFactory.of("L64X128MixRandom").create(seed);
            } catch (Exception e) {
                // If the specific generator isn't available (e.g., older JDK or custom build without it)
                // fall back to the default generator factory and create an instance with the seed.
                System.err.println("Failed to get L64X128MixRandom, falling back to default factory for seeded instance. Error: " + e.getMessage());
                randomGenerator = RandomGeneratorFactory.getDefault().create(seed); // Corrected fallback
            }
        }
    }


    @Benchmark
    public void javaRandomNextLong(JavaRandomState state, Blackhole bh) {
        bh.consume(state.random.nextLong());
    }

    @Benchmark
    public void splittableRandomNextLong(SplittableRandomState state, Blackhole bh) { // Added benchmark
        bh.consume(state.splittableRandom.nextLong());
    }

    @Benchmark
    public void threadLocalRandomNextLong(Blackhole bh) {
        bh.consume(ThreadLocalRandom.current().nextLong());
    }

    @Benchmark
    public void randomGeneratorL64X128MixRandomNextLong(RandomGeneratorState state, Blackhole bh) {
        bh.consume(state.randomGenerator.nextLong());
    }

    @Benchmark
    public void xoroshiro128ppRandom(XoroshiroState state, Blackhole bh) {
        final long result = Long.rotateLeft(state.s0 + state.s1, 17) + state.s0;
        final long currentS1 = state.s1 ^ state.s0;
        state.s0 = Long.rotateLeft(state.s0, 49) ^ currentS1 ^ (currentS1 << 21);
        state.s1 = Long.rotateLeft(currentS1, 28);
        bh.consume(result);
    }

    @Benchmark
    public void benchmarkBiski64(BiskiState state, Blackhole bh) {
        long output = state.mix + state.loopMix;
        long oldLoopMix = state.loopMix;
        state.loopMix = state.fastLoop ^ state.mix;
        state.mix = Long.rotateLeft(state.mix, 16) + Long.rotateLeft(oldLoopMix, 40);
        state.fastLoop += 0x9999999999999999L;
        bh.consume(output);
    }

    @Benchmark
    public void xoshiro256plusplusRandom(Xoshiro256State state, Blackhole bh) {
        final long result = Long.rotateLeft(state.s1 * 5, 7) * 9;
        final long t = state.s1 << 17;
        state.s2 ^= state.s0;
        state.s3 ^= state.s1;
        state.s1 ^= state.s2; // Corrected: was s2, now state.s2
        state.s0 ^= state.s3; // Corrected: was s3, now state.s3
        state.s2 ^= t;
        state.s3 = Long.rotateLeft(state.s3, 45);
        bh.consume(result);
    }

    public static void main(String[] args) throws Exception {
        org.openjdk.jmh.Main.main(args);
    }
}
