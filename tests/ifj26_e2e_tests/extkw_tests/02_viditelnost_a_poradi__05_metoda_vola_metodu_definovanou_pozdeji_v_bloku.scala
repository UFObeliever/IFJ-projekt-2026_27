import ifj26.*

extension (v: Int)
  def a: Int = v.b + 1
  def b: Int = v * 10

@main
def main(): Unit =
  val x = 2
  val r = x.a
  print(r)
