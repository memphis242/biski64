import java.math.BigInteger;

/**
 * Biski64 is a high-performance pseudo-random number generator (PRNG)
 * with a guaranteed minimum period of 2^64. It is designed for
 * non-cryptographic applications where speed and statistical quality are important.
 * <p>
 * **Important Note on Thread Safety:** Instances of this class are NOT thread-safe.
 * If used in a multi-threaded environment, each thread must have its own distinct
 * instance of {@code Biski64}.
 */
public class Biski64
	{
	protected long mix;
	protected long loopMix;
	protected long fastLoop;


	/**
	 * Creates a new Biski64 generator. The seed is initialized using
	 * a value derived from {@code System.nanoTime()}.
	 */
	public Biski64()
		{
		setSeed( System.nanoTime() );
		}



	/**
	 * Creates a new Biski64 generator for a given thread. The seed is initialized using
	 * a value derived from {@code System.nanoTime()}.
	 *
	 * @param threadIndex the index of the thread (starting at 0)
	 * @param totalNumThreads the total number of threads
	 */
	public Biski64( int threadIndex, int totalNumThreads)
		{
		long initialSeed = System.nanoTime() ^ Thread.currentThread().getId() ^ (((long)threadIndex << 16) | totalNumThreads);

		mix = splitMix64( initialSeed );
		loopMix = splitMix64( mix );

		initForThread( threadIndex, totalNumThreads );
		}


	/**
	 * Creates a new Biski64 generator for a given thread. The seed is initialized using
	 * a value derived from {@code System.nanoTime()}.
	 *
	 * @param threadIndex the index of the thread (starting at 0)
	 * @param totalNumThreads the total number of threads
	 * @param seed the initial seed
	 */
	public Biski64( int threadIndex, int totalNumThreads, long seed)
		{
		mix = splitMix64( seed );
		loopMix = splitMix64( mix );

		initForThread( threadIndex, totalNumThreads );
		}


	private void initForThread( int threadIndex, int totalNumThreads)
		{
		final BigInteger FULL_UNSIGNED_64_BIT_RANGE = new BigInteger("FFFFFFFFFFFFFFFFFF", 16);

		if (totalNumThreads == 1)
			this.fastLoop = splitMix64(this.loopMix);
		else
			{
			BigInteger bigNumThreads = BigInteger.valueOf( totalNumThreads );

			// Calculate the increment per stream: (2^64 - 1) / numThreads
			BigInteger incrementPerStream = FULL_UNSIGNED_64_BIT_RANGE.divide( bigNumThreads );

			// Calculate the base fastLoop for this stream: threadIndex * incrementPerStream
			BigInteger baseFastLoopBigInt = incrementPerStream.multiply( BigInteger.valueOf( threadIndex ) );

			// Convert back to long. Since baseFastLoopBigInt should be within 0 to 2^64-1,
			// longValue() will give the correct bit pattern.
			this.fastLoop = baseFastLoopBigInt.longValue();
			}
		}

	/**
	 * Reseeds this generator. The three internal state variables are
	 * derived from the given seed using a SplitMix64 algorithm,
	 * followed by a warm-up period to further diffuse the state.
	 *
	 * @param seed the initial seed
	 */
	public void setSeed( long seed )
		{
		fastLoop = splitMix64( seed );
		mix = splitMix64( fastLoop );
		loopMix = splitMix64( mix );
		}


	/**
	 * A SplitMix64 helper function to scramble and distribute seed bits.
	 *
	 * @param z The input value.
	 * @return A pseudo-random long derived from the input.
	 */
	public static long splitMix64( long z )
		{
		z = ( z ^ ( z >>> 30 ) ) * 0xbf58476d1ce4e5b9L;
		z = ( z ^ ( z >>> 27 ) ) * 0x94d049bb133111ebL;
		return z ^ ( z >>> 31 );
		}


	/**
	 * Returns the next pseudorandom {@code long} value from this generator's sequence.
	 * The general contract of {@code nextLong} is that one {@code long}
	 * value is pseudorandomly generated and returned.
	 *
	 * @return the next pseudorandom {@code long} value
	 */
	public long nextLong()
		{
		final long output = this.mix + this.loopMix;
		final long oldLoopMix = this.loopMix;

		this.loopMix = this.fastLoop ^ this.mix;
		this.mix = Long.rotateLeft( this.mix, 16 ) + Long.rotateLeft( oldLoopMix, 40 );
		this.fastLoop += 0x9999999999999999L;

		return output;
		}


	/**
	 * Returns a pseudorandom {@code double} value between 0.0 (inclusive) and 1.0 (exclusive).
	 *
	 * @return a pseudorandom {@code double} value between 0.0 and 1.0
	 */
	public double random()
		{
		// Uses the top 53 bits from nextLong() for double precision
		return ( nextLong() >>> 11 ) * ( 1.0 / ( 1L << 53 ) );
		}


	/**
	 * Returns a pseudorandom {@code int} value between 0 (inclusive)
	 * and the specified value (inclusive), without modulo bias.
	 *
	 * @param maxInclusive the maximum inclusive bound. Must be non-negative.
	 * @return a pseudorandom {@code int} value between 0 and {@code maxInclusive} (inclusive)
	 * @throws IllegalArgumentException if {@code maxInclusive} is negative
	 */
	public int random( int maxInclusive )
		{
		if ( maxInclusive < 0 )
			throw new IllegalArgumentException( "maxInclusive must be non-negative" );

		if ( maxInclusive == 0 )
			return 0;

		long N = (long) maxInclusive + 1;
		long bits;
		long val;
		do
			{
			bits = nextLong() & Long.MAX_VALUE; // Ensure non-negative
			val = bits % N;
			}
		while ( bits - val + ( N - 1 ) < 0 ); // Debiasing loop

		return (int) val;
		}


	/**
	 * Returns a pseudorandom {@code boolean} value.
	 *
	 * @return a pseudorandom {@code boolean} value
	 */
	public boolean flipCoin()
		{
		return (nextLong() & 1L) == 0L;
		}


	/**
	 * Returns a pseudorandom {@code double} value selected from a
	 * Gaussian (normal) distribution with mean 0.0 and standard deviation 1.0.
	 *
	 * @return a pseudorandom {@code double} from a standard normal distribution
	 */
	public double randomGaussian()
		{
		// Uses the Box-Muller transform
		double v1, v2, s;
		do
			{
			v1 = 2.0 * random() - 1.0; // Between -1.0 and 1.0
			v2 = 2.0 * random() - 1.0; // Between -1.0 and 1.0
			s = v1 * v1 + v2 * v2;
			}
		while ( s >= 1.0 || s == 0.0 );

		double multiplier = Math.sqrt( ( -2.0 * Math.log( s ) ) / s );

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
	public String randomHexString( int length )
		{
		if ( length < 0 )
			throw new IllegalArgumentException( "length must be non-negative" );

		if ( length == 0 )
			return "";

		StringBuilder sb = new StringBuilder( length );
		while ( sb.length() < length )
			{
			// Generate a random int
			long r = nextLong();

			// toHexString will generation up to 16 hex digits (but the top ones might be zero)
			String hex = String.format("%016x", r);
			sb.append( hex );
			}

		// Ensure exact length, trim if necessary.
		if ( sb.length() > length )
			return sb.toString().substring( 0, length );

		return sb.toString();
		}
	}
