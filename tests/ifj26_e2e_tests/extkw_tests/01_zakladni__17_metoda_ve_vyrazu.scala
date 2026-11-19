import ifj26.*

extension (v: Int)
  def twice: Int = v * 2

@main
def main(): Unit =
  val x = 6
  val r = x.twice + x.twice * 2
  print(r)
