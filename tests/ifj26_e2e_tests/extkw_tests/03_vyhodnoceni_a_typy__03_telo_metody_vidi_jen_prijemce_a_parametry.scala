import ifj26.*

extension (v: Int)
  def f(a: Int): Int =
    val t = v * 10
    t + a

@main
def main(): Unit =
  val x = 3
  val r = x.f(4)
  print(r)
