import ifj26.*

extension (d: Double)
  def rounded: Int = (d + 0.5).floor.toInt

@main
def main(): Unit =
  val a = 2.4
  val b = 2.5
  val r1 = a.rounded
  val r2 = b.rounded
  print(r1)
  print(r2)
