import ifj26.*

def digitSum(n: Int): Int =
  var m = n
  var s = 0
  while m > 0 do
    val d = m - m / 10 * 10
    s = s + d
    m = m / 10
  return s

@main def main(): Unit =
  val r = digitSum(987654321)
  print(r)
  print("\n")
