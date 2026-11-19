import ifj26.*

@main
def main(): Unit =
  for
    i <- 1 to 3 // generátor
    if i != 2   // stráž
  do
    print(i)
