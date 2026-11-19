import ifj26.*

def dbl(x: Int): Int = x * 2

extension (v: Int)
  def twice: Int = v * 2

@main
def main(): Unit =
  print(toStr(dbl(3).twice))
