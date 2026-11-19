import ifj26.*

extension (v: Int)
  def f(a: Int): Int = v + a

@main
def main(): Unit =
  val a = 100
  val x = 1
  val r = x.f(5)
  print(r)
