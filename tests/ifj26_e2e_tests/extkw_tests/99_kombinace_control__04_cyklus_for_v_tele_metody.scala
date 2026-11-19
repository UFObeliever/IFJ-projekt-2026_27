import ifj26.*

extension (n: Int)
  def countdown: Unit =
    for i <- 1 to n do
      print(i)

@main
def main(): Unit =
  val x = 3
  x.countdown
