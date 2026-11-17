import ifj26.*

extension (b: Boolean)
  def num: Int =
    if b then
      return 1
    else
      return 0

@main
def main(): Unit =
  val x = true
  val y = false
  val a = x.num
  val c = y.num
  print(a)
  print(c)
