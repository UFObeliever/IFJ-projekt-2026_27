import ifj26.*

extension (n: Int)
  def isPositive: Boolean = n > 0

@main
def main(): Unit =
  val a = 5
  val b = 0 - 5
  if a.isPositive then
    print("a")
  if b.isPositive then
    print("b")
  print("|")
