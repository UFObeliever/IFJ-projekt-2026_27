import ifj26.*

extension (v: Int)
  def f()(b: Int): Int = v * 10 + b

@main
def main(): Unit =
  val x = 4
  val r = x.f()(5)
  print(r)
