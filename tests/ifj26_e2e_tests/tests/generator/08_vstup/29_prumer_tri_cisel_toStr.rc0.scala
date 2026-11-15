import ifj26.*

@main def main(): Unit =
  val n = readInt()
  var i = 0
  var sum = 0
  while i < n do
    val x = readInt()
    sum = sum + x
    i = i + 1
  val sd: Double = sum
  val nd: Double = n
  val avg = sd / nd
  val t = toStr(avg)
  print(t)
  print("\n")
