import ifj26.*

extension (v: Int)
  def twice: Int = v * 2

@main
def main(): Unit =
  val n = 6
  val s = n.twice.toString
  print(s)
