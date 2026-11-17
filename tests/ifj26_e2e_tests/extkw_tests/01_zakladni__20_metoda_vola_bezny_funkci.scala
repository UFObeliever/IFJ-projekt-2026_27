import ifj26.*

def dbl(x: Int): Int = x * 2

extension (v: Int)
  def viaDbl: Int = dbl(v)

@main
def main(): Unit =
  val x = 5
  val r = x.viaDbl
  print(r)
