use std::io;
mod external;

fn main() {
    external::help();
    let mut input = String::new();
    match io::stdin().read_line(&mut input) {
        Ok(_) => {
            println!("hello {input}");
        }
        Err(error) => println!("error: {error}"),
    }
}
