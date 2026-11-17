import ifj26.*

extension (v: Int)
  def clamp(lo: Int, hi: Int): Int =
    if v < lo then
      return lo
    else if v > hi then
      return hi
    else
      return v

@main
def main(): Unit =
  val a = 15
  val b = 0 - 5
  val c = 5
  val r1 = a.clamp(0, 10)
  val r2 = b.clamp(0, 10)
  val r3 = c.clamp(0, 10)
  print(r1)
  print(" ")
  print(r2)
  print(" ")
  print(r3)
