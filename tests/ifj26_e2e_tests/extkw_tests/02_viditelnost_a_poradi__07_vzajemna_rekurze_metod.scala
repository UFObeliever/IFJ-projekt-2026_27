import ifj26.*

extension (n: Int)
  def evenFlag: Int =
    if n == 0 then
      return 1
    else
      return (n - 1).oddFlag
  def oddFlag: Int =
    if n == 0 then
      return 0
    else
      return (n - 1).evenFlag

@main
def main(): Unit =
  val a = 10
  val b = 7
  val r1 = a.evenFlag
  val r2 = b.evenFlag
  print(r1)
  print(r2)
