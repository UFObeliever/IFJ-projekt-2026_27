import ifj26.*

@main def main(): Unit =
  val a = strLength("hello")
  val b = strLength("")
  val c = strLength("a\nb")
  val s = "ab" + "cde"
  val d = strLength(s)
  print(a)
  print("\n")
  print(b)
  print("\n")
  print(c)
  print("\n")
  print(d)
  print("\n")
