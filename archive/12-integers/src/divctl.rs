// divctl.rs: control in Rust: the language's own division, over divbench2's operands (divbench2 dump), timed the same
// way (reps of at least 50 ms, 15 reps, median per division) and checksummed the same way.
//   divctl_rs <operand file> <set>     set 32 and 64: u64 a / b; set 128: u128 a / b
use std::time::Instant;

const N: usize = 1024;

struct D { b: [u64; N], alo: [u64; N], ahi: [u64; N], a2lo: [u64; N], a2hi: [u64; N], r: [u64; N], r2: [u64; N] }

#[inline(never)]
fn c64(d: &mut D, k: u64) {
    for _ in 0..k {
        for j in 0..N {
            d.r[j] = d.alo[j] / d.b[j];
            d.r2[j] = d.a2lo[j] / d.b[j];
        }
        std::hint::black_box(&mut d.r);
        std::hint::black_box(&mut d.r2);
    }
}

#[inline(never)]
fn c128(d: &mut D, k: u64) {
    for _ in 0..k {
        for j in 0..N {
            let b = d.b[j] as u128;
            d.r[j] = ((((d.ahi[j] as u128) << 64) | d.alo[j] as u128) / b) as u64;
            d.r2[j] = ((((d.a2hi[j] as u128) << 64) | d.a2lo[j] as u128) / b) as u64;
        }
        std::hint::black_box(&mut d.r);
        std::hint::black_box(&mut d.r2);
    }
}

fn main() {
    let args: Vec<String> = std::env::args().collect();
    let text = std::fs::read_to_string(&args[1]).unwrap();
    let set: u32 = args[2].parse().unwrap();
    let mut d = Box::new(D { b: [0; N], alo: [0; N], ahi: [0; N], a2lo: [0; N], a2hi: [0; N], r: [0; N], r2: [0; N] });
    let mut n = 0;
    for (j, line) in text.lines().enumerate() {
        let v: Vec<u64> = line.split_whitespace().map(|x| x.parse().unwrap()).collect();
        d.b[j] = v[0]; d.alo[j] = v[1]; d.ahi[j] = v[2]; d.a2lo[j] = v[3]; d.a2hi[j] = v[4];
        n += 1;
    }
    assert_eq!(n, N);
    let f: fn(&mut D, u64) = if set == 128 { c128 } else { c64 };
    let mut k = 1u64;
    loop {
        let t0 = Instant::now();
        f(&mut d, k);
        if t0.elapsed().as_nanos() >= 50_000_000 { break; }
        k *= 2;
    }
    let mut ts = Vec::new();
    for _ in 0..15 {
        let t0 = Instant::now();
        f(&mut d, k);
        ts.push(t0.elapsed().as_nanos() as f64 / (k as f64 * 2.0 * N as f64));
    }
    ts.sort_by(|a, b| a.partial_cmp(b).unwrap());
    let mut h = 0u64;
    for j in 0..N { h = (h ^ d.r[j] ^ (d.r2[j] << 1)).wrapping_mul(0x9E3779B97F4A7C15); }
    println!("rust {} {:.4} {:.4} {:016X}", set, ts[7], (ts[14] - ts[0]) / ts[7], h);
}
