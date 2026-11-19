import ifj26.*

@main
def main(): Unit =
  val n = 6
  for
    i <- 0 until n
    if i != 0
  do
    print(i)
