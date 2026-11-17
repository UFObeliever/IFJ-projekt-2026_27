import ifj26.*

def plusOne(x: Int): Int = x + 1

extension (v: Int)
  def plusTwo: Int = v + 2

@main
def main(): Unit =
  val x = 1
  val a = plusOne(x)
  val b = x.plusTwo
  print(a)
  print(b)
