import ifj26.*

def t(tag: String, v: Boolean): Boolean =
  print(tag)
  v

@main
def main(): Unit =
  val r = t("a", true) || t("b", true) && t("c", true)
  print("|")
  print(r)
