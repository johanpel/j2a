use arrow::datatypes::{DataType, Field, Schema};
use rand::rngs::SmallRng;
use rand::{Rng, SeedableRng};
use serde::{Deserialize, Serialize};
use serde_json::Result;
use std::sync::Arc;
use std::time::{Duration, Instant};

use nom::bytes::complete::tag;
use nom::character::complete::digit1;
use nom::combinator::map;
use nom::multi::{many0, many1, separated_list0};
use nom::number::complete::u64;
use nom::sequence::{pair, tuple};
use nom::IResult;
use std::str::FromStr;

fn bs_header(s: &str) -> IResult<&str, &str> {
    tag("{\"voltage\":[")(s)
}

fn bs_array(s: &str) -> IResult<&str, Vec<u64>> {
    map(separated_list0(tag(","), digit1), |vec: Vec<&str>| {
        vec.into_iter()
            .map(|val: &str| u64::from_str(val).unwrap())
            .collect()
    })(s)
}

fn bs_footer(s: &str) -> IResult<&str, &str> {
    tag("]}")(s)
}

fn bs_parser(s: &str) -> IResult<&str, Vec<u64>> {
    map(tuple((bs_header, bs_array, bs_footer)), |(_, a, _)| a)(s)
}

fn bs_many_parser(s: &str) -> IResult<&str, Vec<u64>> {
    map(
        separated_list0(tag("\n"), bs_parser),
        |vecs: Vec<Vec<u64>>| vecs.into_iter().flatten().collect(),
    )(s)
}

#[derive(Serialize, Deserialize)]
struct BatteryStatus {
    voltage: Vec<u64>,
}

struct BatteryRngDimensions {
    num_values_min: usize,
    num_values_max: usize,
    val_min: u64,
    val_max: u64,
}

impl Default for BatteryRngDimensions {
    fn default() -> Self {
        BatteryRngDimensions {
            num_values_min: 1,
            num_values_max: 16,
            val_min: 0,
            val_max: 4095,
        }
    }
}

fn gen_battery_jsons(num_jsons: usize, dims: BatteryRngDimensions) -> String {
    let mut rng = SmallRng::seed_from_u64(0);
    let mut result = String::new();
    for i in 0..num_jsons {
        result.push_str(gen_battery_json(&mut rng, &dims).as_str())
    }
    result
}

fn gen_battery_json(rng: &mut impl Rng, dims: &BatteryRngDimensions) -> String {
    let mut result = String::new();
    result.push_str("{\"voltage\":[");
    let num_values = rng.gen_range(dims.num_values_min..dims.num_values_max);
    for i in 0..num_values {
        result.push_str(format!("{}", rng.gen_range(dims.val_min..dims.val_max)).as_str());
        if i != num_values - 1 {
            result.push(',');
            let str = "library ieee";
        }
    }
    result.push_str("]}\n");
    result
}

fn battery_schema() -> Schema {
    Schema::new(vec![Field::new(
        "voltage",
        DataType::List(Box::new(Field::new("item", DataType::UInt64, false))),
        false,
    )])
}

fn main() -> Result<()> {
    const NUM_JSONS: usize = 2 * 1024 * 1024;
    let jsons = gen_battery_jsons(NUM_JSONS, BatteryRngDimensions::default());

    let mut json = arrow::json::Reader::new(jsons.as_bytes(), Arc::new(battery_schema()), NUM_JSONS, None);
    let mut start = Instant::now();
    let batch = json.next().unwrap().unwrap();
    let mut duration = start.elapsed();
    println!("Rust, Arrow: {:?}s", duration.as_secs_f64());
    dbg!(batch.num_rows());

    let mut start = Instant::now();
    let nom_result = bs_many_parser(jsons.as_str());
    let mut duration = start.elapsed();
    println!("Rust, nom: {:?}s", duration.as_secs_f64());

    dbg!(nom_result.unwrap_or_default().1.len());
    Ok(())
}
