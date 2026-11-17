import ifj26.*

extension (n: Int)
  def sumTo: Int =
    var s = 0
    var i = 1
    while i <= n do
      s = s + i
      i = i + 1
    s

@main
def main(): Unit =
  val x = 10
  val r = x.sumTo
  print(r)
