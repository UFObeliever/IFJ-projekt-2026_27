import ifj26.*

extension (v: Int)
  def twice: Int = v * 2
  def quad: Int = v.twice.twice

@main
def main(): Unit =
  val x = 3
  val r = x.quad
  print(r)
