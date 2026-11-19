import ifj26.*

extension (v: Int)
  def twice: Int = v * 2

def add(a: Int, b: Int): Int = a + b

@main
def main(): Unit =
  val x = 2
  print(toStr(add(x.twice, x.twice)))
