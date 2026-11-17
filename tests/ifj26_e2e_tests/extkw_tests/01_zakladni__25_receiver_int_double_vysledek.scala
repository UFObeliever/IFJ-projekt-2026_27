import ifj26.*

extension (v: Int)
  def half: Double = v / 2.0

@main
def main(): Unit =
  val x = 5
  val r = x.half
  val s = toStr(r)
  print(s)
