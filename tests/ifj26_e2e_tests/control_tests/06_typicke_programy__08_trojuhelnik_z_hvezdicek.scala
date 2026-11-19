import ifj26.*

@main
def main(): Unit =
  for i <- 1 to 3 do
    for j <- 1 to i do print("*")
    print("\n")
