import ifj26.*

extension (v: Int)
  def twice: Int = v * 2

@main
def main(): Unit =
  val x = 2
  for i <- 1 to x.twice do print(i)
