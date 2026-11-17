import ifj26.*

extension (v: Int)
  def tagA: Int =
    print("a")
    v
  def tagB: Int =
    print("b")
    v

@main
def main(): Unit =
  val x = 1
  val r = x.tagA + x.tagB * x.tagA
  print("|")
  print(r)
