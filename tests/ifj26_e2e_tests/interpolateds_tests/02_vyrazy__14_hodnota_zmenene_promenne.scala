import ifj26.*

@main
def main(): Unit =
  var n = 1
  val before = s"n=$n"
  n = n + 1
  val after = s"n=$n"
  print(before)
  print(",")
  print(after)
