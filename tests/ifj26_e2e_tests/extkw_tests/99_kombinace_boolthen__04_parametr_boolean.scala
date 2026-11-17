import ifj26.*

extension (n: Int)
  def pick(flag: Boolean): Int =
    if flag then
      return n
    else
      return 0

@main
def main(): Unit =
  val x = 7
  val a = x.pick(true)
  val b = x.pick(false)
  print(a)
  print(b)
