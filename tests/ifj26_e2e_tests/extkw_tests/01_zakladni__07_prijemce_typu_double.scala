import ifj26.*

extension (d: Double)
  def half: Double = d / 2.0

@main
def main(): Unit =
  val d = 5.0
  val r = d.half
  val s = toStr(r)
  print(s)
