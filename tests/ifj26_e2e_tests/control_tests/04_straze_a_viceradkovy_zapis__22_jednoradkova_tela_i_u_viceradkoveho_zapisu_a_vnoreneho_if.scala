import ifj26.*

@main
def main(): Unit =
  for
    i <- 1 to 4
  do
    if i / 2 * 2 == i then print("e") else print("o")
