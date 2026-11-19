import ifj26.*

extension (d: Double)
  def scale(f: Double): Double = d * f

@main
def main(): Unit =
  val d = 1.25
  val r = d.scale(2)
  val s = toStr(r)
  print(s)
