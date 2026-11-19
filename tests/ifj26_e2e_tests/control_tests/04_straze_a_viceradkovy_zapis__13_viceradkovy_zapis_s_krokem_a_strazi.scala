import ifj26.*

@main
def main(): Unit =
  for
    i <- 10 to 1 by -3
    if i != 7
  do
    print(i)
    print(" ")
