import ifj26.*

extension (n: Int)
  def isEven: Boolean = n / 2 * 2 == n

@main
def main(): Unit =
  for i <- 1 to 6 if i.isEven do print(i)
