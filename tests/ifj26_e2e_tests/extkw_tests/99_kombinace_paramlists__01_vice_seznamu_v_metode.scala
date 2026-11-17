import ifj26.*

extension (v: Int)
  def add(a: Int)(b: Int): Int = v + a + b

@main
def main(): Unit =
  val x = 1
  val r = x.add(10)(100)
  print(r)
