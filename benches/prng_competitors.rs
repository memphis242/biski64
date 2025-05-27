use criterion::{black_box, criterion_group, criterion_main, Criterion};
use biski64::Biski64Rng;
use rand::prelude::*;
use rand_xoshiro::{Xoroshiro128PlusPlus, Xoshiro256PlusPlus};
use std::time::Duration;

fn prng_benchmark_suite(c: &mut Criterion) {
    let mut group = c.benchmark_group("ns_per_call");
    group.measurement_time(Duration::from_secs(15));
    group.sample_size(1000000);
    group.significance_level(0.1).confidence_level(0.95);
    group.nresamples(1000);

    // --- Benchmark biski64 ---
    group.bench_function("biski64", |b| {
        let mut rng = Biski64Rng::seed_from_u64(12345);
        b.iter(|| {
            black_box(rng.next_u64());
        })
    });

    // --- Benchmark xoshiro256++ ---
    group.bench_function("xoshiro256++", |b| {
        let mut rng = Xoshiro256PlusPlus::seed_from_u64(12345);
        b.iter(|| {
            black_box(rng.next_u64());
        })
    });

    // --- Benchmark xoroshiro128++ ---
    group.bench_function("xoroshiro128++", |b| {
        let mut rng = Xoroshiro128PlusPlus::seed_from_u64(12345);
        b.iter(|| {
            black_box(rng.next_u64());
        })
    });

    group.finish();
}

criterion_group!(benches, prng_benchmark_suite);
criterion_main!(benches);
