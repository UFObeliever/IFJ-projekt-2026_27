import ifj26.*

@main def main(): Unit =
  val limit = 10
  for
    i <- 0 until limit
    if i != 3
    if i < 8
  do
    print(i)
  print("\n")
