import ifj26.*

extension (v: Int)
  def twice: Int = v * 2

@main
def main(): Unit =
  val x = 4
  val r: Int = x.twice
  print(r)
