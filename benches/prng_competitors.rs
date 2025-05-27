use criterion::{black_box, criterion_group, criterion_main, Criterion};
// Note the change here to use the new package name 'biski64'
use biski64::Biski64Rng; 
use rand::prelude::*;
use rand_wyrand::WyRand;
use rand_xoshiro::Xoroshiro128PlusPlus;
use std::time::Duration;

const BATCH_SIZE: u64 = 1000;

fn prng_benchmark_suite(c: &mut Criterion) {
    let mut group = c.benchmark_group("ns_per_call");
    group.measurement_time(Duration::from_secs(15));
    group.sample_size(200);

    // --- Benchmark biski64 ---
    group.bench_function("biski64", |b| {
        // This is the main change! We now use the standard seed_from_u64.
        let mut rng = Biski64Rng::seed_from_u64(12345);
        b.iter(|| {
            for _ in 0..BATCH_SIZE {
                black_box(rng.next_u64());
            }
        })
    });

    // --- Benchmark wyrand ---
    group.bench_function("wyrand", |b| {
        let mut rng = WyRand::from_entropy();
        b.iter(|| {
            for _ in 0..BATCH_SIZE {
                black_box(rng.next_u64());
            }
        })
    });

    // --- Benchmark xoroshiro128++ ---
    group.bench_function("xoroshiro128++", |b| {
        let mut rng = Xoroshiro128PlusPlus::from_entropy();
        b.iter(|| {
            for _ in 0..BATCH_SIZE {
                black_box(rng.next_u64());
            }
        })
    });

    group.finish();
}

criterion_group!(benches, prng_benchmark_suite);
criterion_main!(benches);
