import ifj26.*

extension (v: Int)
  def bump: Int =
    var t = v
    t = t + 100
    t

@main
def main(): Unit =
  var x = 1
  val a = x.bump
  x = 5
  val b = x.bump
  print(a)
  print(",")
  print(b)
  print(",")
  print(x)
