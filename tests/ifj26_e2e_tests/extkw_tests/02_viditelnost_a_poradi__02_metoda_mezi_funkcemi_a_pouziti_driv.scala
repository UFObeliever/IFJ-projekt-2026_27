import ifj26.*

def useIt(x: Int): Int =
  val r = x.twice
  r + 1

extension (v: Int)
  def twice: Int = v * 2

@main
def main(): Unit =
  val r = useIt(5)
  print(r)
