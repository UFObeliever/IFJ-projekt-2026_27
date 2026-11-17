import ifj26.*

@main
def main(): Unit =
  val x = 6
  val y = x.twice
  print(y)

extension (v: Int)
  def twice: Int = v * 2
