import ifj26.*

@main
def main(): Unit =
  val skip = 3
  for i <- 1 to 5 if i != skip do print(i)
