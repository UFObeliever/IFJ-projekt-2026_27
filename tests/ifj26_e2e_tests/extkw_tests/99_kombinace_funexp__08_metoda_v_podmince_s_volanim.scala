import ifj26.*

extension (v: Int)
  def twice: Int = v * 2

def dbl(x: Int): Int = x * 2

@main
def main(): Unit =
  val x = 3
  if dbl(x).twice > 10 then
    print("y")
  else
    print("n")
