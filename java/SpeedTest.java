import java.util.Random;

public class SpeedTest extends Biski64
	{
	private static final int NUM_SAMPLES = 1_000_000_000; // 1 billion samples


	// State for xoroshiro128++
	static long xoroshiroS0 = 0x1234567890ABCDEFL;
	static long xoroshiroS1 = 0xDEFCBA0987654321L;

	// State for xoshiro256++
	static long[] s = { 0x1234567890ABCDEFL, 0xDEFCBA0987654321L, 0xABCDEF1234567890L, 0x876543210FEDCBAL };


	public static long xoroshiro128pp()
		{
		long result = Long.rotateLeft( xoroshiroS0 + xoroshiroS1, 17 ) + xoroshiroS0;

		long s1_temp = xoroshiroS1 ^ xoroshiroS0;
		xoroshiroS0 = Long.rotateLeft( xoroshiroS0, 49 ) ^ s1_temp ^ ( s1_temp << 21 );
		xoroshiroS1 = Long.rotateLeft( s1_temp, 28 );

		return result;
		}


	public static long xoshiro256pp()
		{
		final long result = Long.rotateLeft( s[0] + s[3], 23 ) + s[0];

		final long t = s[1] << 17;

		s[2] ^= s[0];
		s[3] ^= s[1];
		s[1] ^= s[2];
		s[0] ^= s[3];

		s[2] ^= t;

		s[3] = Long.rotateLeft( s[3], 45 );

		return result;
		}


	private void benchmark()
		{
		long startTime = System.currentTimeMillis();
		for ( int i = 0; i < NUM_SAMPLES; i++ )
			nextLong();
		double bitsTime = System.currentTimeMillis() - startTime;


		startTime = System.currentTimeMillis();
		for ( int i = 0; i < NUM_SAMPLES; i++ )
			xoroshiro128pp();
		double xoro128Time = System.currentTimeMillis() - startTime;

		startTime = System.currentTimeMillis();
		for ( int i = 0; i < NUM_SAMPLES; i++ )
			xoshiro256pp();
		double xoshiro256Time = System.currentTimeMillis() - startTime;


		Random javaRandom = new Random( 123L ); // Using a fixed seed for reproducibility
		startTime = System.currentTimeMillis();
		for ( int i = 0; i < NUM_SAMPLES; i++ )
			javaRandom.nextLong(); // Using nextLong for a more direct comparison
		double javaTime = System.currentTimeMillis() - startTime;

		System.out.println( "Verification sum: " + ( this.loopMix + xoroshiroS1 + s[0] + javaRandom.nextLong() ) );

		System.out.println( "Generated " + NUM_SAMPLES + " random numbers using each generator." );
		System.out.println( "Time for biski64.nextLong(): " + bitsTime + " ms" );
		System.out.println( "Time for xoroshiro128++:  " + xoro128Time + " ms" );
		System.out.println( "Time for xoshiro256++:    " + xoshiro256Time + " ms" );
		System.out.println( "Time for Java Random.nextLong(): " + javaTime + " ms" );
		System.out.println();

		System.out.println( "biski64.nextLong() is " + (int) ( 100.0 * ( javaTime / bitsTime - 1.0) ) + "% faster than Java Random (nextLong)" );
		System.out.println( "biski64.nextLong() is " + (int) ( 100.0 * ( xoro128Time / bitsTime - 1) ) + "% faster than xoroshiro128++" );
		System.out.println( "biski64.nextLong() is " + (int) (100.0 * ( xoshiro256Time / bitsTime - 1) ) + "% faster than xoshiro256++" );
		}


	public static void main( String[] args )
		{
		SpeedTest speedTest = new SpeedTest();

		speedTest.benchmark();
		}
	}
