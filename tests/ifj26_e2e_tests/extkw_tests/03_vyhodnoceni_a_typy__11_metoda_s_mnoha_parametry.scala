import ifj26.*

extension (v: Int)
  def f(a: Int, b: Int, c: Int): Int = v * 1000 + a * 100 + b * 10 + c

@main
def main(): Unit =
  val x = 1
  val r = x.f(2, 3, 4)
  print(r)
