import ifj26.*

extension (v: Int)
  def twice: Int = v * 2

@main
def main(): Unit =
  val x = 6
  val l = x.twice.toString.length
  print(l)
