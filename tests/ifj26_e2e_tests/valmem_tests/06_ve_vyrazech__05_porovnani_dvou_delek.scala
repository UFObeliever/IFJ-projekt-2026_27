import ifj26.*

@main
def main(): Unit =
  val a = "ab"
  val b = "abc"
  if a.length < b.length then
    print("lt")
  else
    print("ge")
